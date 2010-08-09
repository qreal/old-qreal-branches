#include "javaHandler.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtCore/QDebug>

#include <QFileInfo>
#include <QDir>

#include "../../kernel/definitions.h"
#include "../../../qrrepo/repoApi.h"

using namespace qReal;
using namespace generators;

static pjavaLexer lxr;

JavaHandler::JavaHandler(qrRepo::RepoApi const &api)
    : mApi(api)
{
}

//Parsing Java Libraries

QString JavaHandler::parseJavaLibraries(QString const &pathToDir)
{
    qDebug() << "start : pathToDir = " + pathToDir;

    Package javaLibrariesPackage;
    packages.append(javaLibrariesPackage);
    QStringList files = getAllFilesInDirectory(pathToDir, javaLibrariesPackage);

    qDebug() << "finished parsing OK";

    return "";
}

QStringList JavaHandler::getAllFilesInDirectory(QString dir_name, Package &package)
{
    QStringList ret_list;
    QDir dir(dir_name);
    QFileInfoList info_list = dir.entryInfoList();
    if(info_list.size() > 2) {
        QList<QFileInfo>::iterator iter = info_list.begin();
        QString path;
        for (iter = info_list.begin() + 2; iter != info_list.end(); iter++) {
            path = iter->absoluteFilePath();
            if (iter->isDir()) { //it is a directory => creating new package
                QString packageName = path;
                packageName.remove(0, dir_name.length() + 1);
                qDebug() << "packageName = " + packageName;
                Package newPackage;
                newPackage.name = packageName;
                package.packages.append(newPackage);

                ret_list += getAllFilesInDirectory(path, newPackage);
            } else {
                if (path.endsWith(".java")) { //it is a file => creating new structure

                    //-----------------------------------------
                    qDebug() << path;

                    //QString -> char *
                    QByteArray byteArray = path.toLatin1();
                    char * fileName = byteArray.data();

                    javaParser_compilationUnit_return compilationUnit = parseFile((pANTLR3_UINT8)fileName);
                    pANTLR3_BASE_TREE tree = compilationUnit.tree;

                    QStringList attributes = classAttributes(tree);
                    if (!attributes.isEmpty()) {
                        QString structureDeclaration = attributes.takeFirst(); //it is class or interface declaration
//                        qDebug() << "structureDeclaration = " + structureDeclaration;
                        if (structureDeclaration.startsWith("public") && (structureDeclaration.contains("class")
                                || structureDeclaration.contains("public interface"))) {
                            Structure fileStructure(structureDeclaration);

                            Q_FOREACH (QString anAttr, attributes) {
                                if (anAttr.contains("(")) { //method
//                                    qDebug() << "method = " + anAttr;
                                    Method method(anAttr);
                                    fileStructure.methods.append(method);
                                } else { //attribute
//                                    qDebug() << "attribute = " + anAttr;
                                    Attribute attribute(anAttr);
                                    fileStructure.attributes.append(attribute);
                                }
                            }

//                            qDebug() << fileStructure.serializeMe();
                            package.structures.append(fileStructure);
                        }
                    }
                    //---------------------------------------------


                    ret_list.append(path);
                }
            }
        }
    }
    return ret_list;
}

javaParser_compilationUnit_return JavaHandler::parseFile(pANTLR3_UINT8 fileName)
{
    javaParser_compilationUnit_return result;

    // Now we declare the ANTLR related local variables we need.
    // Note that unless you are convinced you will never need thread safe
    // versions for your project, then you should always create such things
    // as instance variables for each invocation.
    // -------------------

    // The ANTLR3 character input stream, which abstracts the input source such that
    // it is easy to provide input from different sources such as files, or
    // memory strings.
    //
    // For an ASCII/latin-1 memory string use:
    //	    input = antlr3NewAsciiStringInPlaceStream (stringtouse, (ANTLR3_UINT64) length, NULL);
    //
    // For a UCS2 (16 bit) memory string use:
    //	    input = antlr3NewUCS2StringInPlaceStream (stringtouse, (ANTLR3_UINT64) length, NULL);
    //
    // For input from a file, see code below
    //
    // Note that this is essentially a pointer to a structure containing pointers to functions.
    // You can create your own input stream type (copy one of the existing ones) and override any
    // individual function by installing your own pointer after you have created the standard
    // version.
    //
    pANTLR3_INPUT_STREAM input;


    // The token stream is produced by the ANTLR3 generated lexer. Again it is a structure based
    // API/Object, which you can customise and override methods of as you wish. a Token stream is
    // supplied to the generated parser, and you can write your own token stream and pass this in
    // if you wish.
    //
    pANTLR3_COMMON_TOKEN_STREAM tstream;

    // The C parser is also generated by ANTLR and accepts a token stream as explained
    // above. The token stream can be any source in fact, so long as it implements the
    // ANTLR3_TOKEN_SOURCE interface. In this case the parser does not return anything
    // but it can of course specify any kind of return type from the rule you invoke
    // when calling it.
    //
    pjavaParser	psr;

    // Create the input stream using the supplied file name
    // (Use antlr3AsciiFileStreamNew for UCS2/16bit input).
    //
    input = antlr3AsciiFileStreamNew(fileName);

    // The input will be created successfully, providing that there is enough
    // memory and the file exists etc
    //
    if ( input == NULL ) {
        qDebug() << "File not found: " + QString(QLatin1String((char *)fileName));
        exit(1);
    }

    // Our input stream is now open and all set to go, so we can create a new instance of our
    // lexer and set the lexer input to our input stream:
    //  (file | memory | ?) --> inputstream -> lexer --> tokenstream --> parser ( --> treeparser )?
    //
    if (lxr == NULL) {
        lxr = javaLexerNew(input);	    // javaLexerNew is generated by ANTLR
    } else {
        lxr->pLexer->setCharStream(lxr->pLexer, input);
    }

    // Need to check for errors
    //
    if (lxr == NULL) {
        qDebug() << "Unable to create the lexer due to malloc() failure1\n";
        exit(ANTLR3_ERR_NOMEM);
    }

    // Our lexer is in place, so we can create the token stream from it
    // NB: Nothing happens yet other than the file has been read. We are just
    // connecting all these things together and they will be invoked when we
    // call the parser rule. ANTLR3_SIZE_HINT can be left at the default usually
    // unless you have a very large token stream/input. Each generated lexer
    // provides a token source interface, which is the second argument to the
    // token stream creator.
    // Note that even if you implement your own token structure, it will always
    // contain a standard common token within it and this is the pointer that
    // you pass around to everything else. A common token as a pointer within
    // it that should point to your own outer token structure.
    //
    tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lxr));

    if (tstream == NULL) {
        qDebug() << "Out of memory trying to allocate token stream\n";
        exit(ANTLR3_ERR_NOMEM);
    }

    // Finally, now that we have our lexer constructed, we can create the parser
    //
    psr = javaParserNew(tstream);  // javaParserNew is generated by ANTLR3

    if (tstream == NULL) {
        qDebug() << "Out of memory trying to allocate parser\n";
        exit(ANTLR3_ERR_NOMEM);
    }

    // We are all ready to go. Though that looked complicated at first glance,
    // I am sure, you will see that in fact most of the code above is dealing
    // with errors and there isn't really that much to do (isn't this always the
    // case in C? ;-).
    //
    // So, we now invoke the parser. All elements of ANTLR3 generated C components
    // as well as the ANTLR C runtime library itself are pseudo objects. This means
    // that they are represented as pointers to structures, which contain any
    // instance data they need, and a set of pointers to other interfaces or
    // 'methods'. Note that in general, these few pointers we have created here are
    // the only things you will ever explicitly free() as everything else is created
    // via factories, that allocate memory efficiently and free() everything they use
    // automatically when you close the parser/lexer/etc.
    //
    // Note that this means only that the methods are always called via the object
    // pointer and the first argument to any method, is a pointer to the structure itself.
    // It also has the side advantage, if you are using an IDE such as VS2005 that can do it,
    // that when you type ->, you will see a list of all the methods the object supports.
    //
    //putc('L', stdout); fflush(stdout);
        //{
        //	ANTLR3_INT32 T;

        //	T = 0;
        //	while	(T != ANTLR3_TOKEN_EOF)
        //	{
        //		T = tstream->tstream->istream->_LA(tstream->tstream->istream, 1);
        //		tstream->tstream->istream->consume(tstream->tstream->istream);
        //		printf("%d %s\n", T,  (psr->pParser->rec->state->tokenNames)[T]);
        //	}
        //}
    tstream->tstream->_LT(tstream->tstream, 1);	// Don't do this normally, just causes lexer to run for timings here
    //putc('P', stdout); fflush(stdout);

    result = psr->compilationUnit(psr);

    putc('*', stdout);
    fflush(stdout);

    // We did not return anything from this parser rule, so we can finish. It only remains
    // to close down our open objects, in the reverse order we created them
    //
//    psr->free(psr);
//    psr = NULL;
//    tstream->free(tstream);
//    tstream = NULL;
//    lxr->free(lxr);
//    lxr = NULL;
//    input->close(input);
//    input = NULL;

    return result;
}

pANTLR3_STRING JavaHandler::className(pANTLR3_BASE_TREE tree)
{
    pANTLR3_STRING  string;
    ANTLR3_UINT32   i;
    ANTLR3_UINT32   n;
    pANTLR3_BASE_TREE   t;
    bool shouldAppend = false;

    if	(tree->children == NULL || tree->children->size(tree->children) == 0)
    {
            return	className(tree);
    }

    /* Need a new string with nothing at all in it.
    */
    string = tree->strFactory->newRaw(tree->strFactory);

    if	(tree->children != NULL)
    {
            n = tree->children->size(tree->children);

            for	(i = 0; i < n; i++)
            {
                    t   = (pANTLR3_BASE_TREE) tree->children->get(tree->children, i);

                    pANTLR3_STRING toString = t->toString(t);
                    if (shouldAppend) {
                        string->appendS(string, toString);
                        return string;
                    }
                    if ( QString(QLatin1String((char*)(toString->chars))) == "class"
                         || QString(QLatin1String((char*)(toString->chars))) == "interface" ) {
                        string->appendS(string, toString);
                        string->append8(string, " ");
                        shouldAppend = true;
                    }
            }
    }

    return  string;
}

pANTLR3_STRING JavaHandler::classAttributesInString(pANTLR3_BASE_TREE tree)
{
    pANTLR3_STRING  string;
    ANTLR3_UINT32   i;
    ANTLR3_UINT32   n;
    pANTLR3_BASE_TREE   t;
    bool shouldAppend = false;

    if	(tree->children == NULL || tree->children->size(tree->children) == 0)
    {
            return	classAttributesInString(tree);
    }

    /* Need a new string with nothing at all in it.
    */
    string = tree->strFactory->newRaw(tree->strFactory);

    if	(tree->children != NULL)
    {
            n = tree->children->size(tree->children);

            for	(i = 0; i < n; i++)
            {
                    t   = (pANTLR3_BASE_TREE) tree->children->get(tree->children, i);

                    if  (i > 0)
                    {
                            string->append8(string, " ");
                    }
                    pANTLR3_STRING toString = t->toString(t);
                    if (shouldAppend) {
                        if (QString(QLatin1String((char *)toString->chars)) != "{"){
                            string->appendS(string, toString);
                        } else {
                            string->append8(string, ";");
                        }
                        string->append8(string, " ");
                    }
                    if (QString(QLatin1String((char *)toString->chars)) == "public" || QString(QLatin1String((char *)toString->chars)) == "protected") {
                        string->appendS(string, toString);
                        string->append8(string, " ");
                        shouldAppend = true;
                    }
                    if (QString(QLatin1String((char *)toString->chars)) == ";" || QString(QLatin1String((char *)toString->chars)) == "{") {
                        shouldAppend = false;
                    }
            }
    }

    return  string;
}

QStringList JavaHandler::classAttributes(pANTLR3_BASE_TREE tree)
{
    QStringList result;

    pANTLR3_STRING attributesANTLR3 = classAttributesInString(tree);
    QString attributes = QString(QLatin1String((char *) attributesANTLR3 -> chars));
    attributes = attributes.simplified();
    result = attributes.split(";");

    QString last = result.takeLast();
    if (last != "") {
        result.append(last);
    }

    return result;
}

//Code Generating

QString JavaHandler::generateToJava(QString const &pathToDir)
{
    mErrorText = "";
    this->pathToDir = pathToDir;

    Id repoId = ROOT_ID;

    if (checkTheModel()) {
        IdList allDiagrams = mApi.children(repoId);
        IdList classDiagrams;

        //separate just class diagrams, because they are the main diagrams, others are connected
        Q_FOREACH (Id const aDiagram, allDiagrams) {
            if (objectType(aDiagram) == "ClassDiagram_ClassDiagramNode") {
                classDiagrams.append(aDiagram);
            }
            if (objectType(aDiagram) == "ActivityDiagram_ActivityDiagramNode") {
                //If there is no connected Class Methods it won't be serialized
                IdList incomingConnections = mApi.incomingConnections(aDiagram);
                if (incomingConnections.isEmpty()) {
                    addError("Unable to serialize object " + objectType(aDiagram) + " with id: " + aDiagram.toString() + ". It is not connected to some class method.");
                }
            }
        }

        Q_FOREACH (Id const classDiagram, classDiagrams) {
            serializeChildren(classDiagram);
        }
    }

    qDebug() << "Done.";
    return mErrorText;
}

bool JavaHandler::checkTheModel()
{
    bool result = true;

    IdList links;

    //class diagram
    IdList aggregations = mApi.elementsByType("ClassDiagram_Aggregation");
    IdList associations = mApi.elementsByType("ClassDiagram_Association");
    IdList classDirectedAssociations = mApi.elementsByType("ClassDiagram_DirectedAssociation");
//    IdList classes = mApi.elementsByType("ClassDiagram_Class");
    IdList classFields = mApi.elementsByType("ClassDiagram_ClassField");
    IdList classMethods = mApi.elementsByType("ClassDiagram_ClassMethod");
//    IdList comments = mApi.elementsByType("ClassDiagram_Comment");
    IdList classCommentLinks = mApi.elementsByType("ClassDiagram_CommentLink");
    IdList compositions = mApi.elementsByType("ClassDiagram_Composition");
    IdList dependencies = mApi.elementsByType("ClassDiagram_Dependency");
    IdList generalizations = mApi.elementsByType("ClassDiagram_Generalization");
//    IdList interfaces = mApi.elementsByType("ClassDiagram_Interface");
    IdList interfaceRealizations = mApi.elementsByType("ClassDiagram_InterfaceRealization");
    IdList realizations = mApi.elementsByType("ClassDiagram_Realization");
//    IdList views = mApi.elementsByType("ClassDiagram_View");

    links.append(aggregations);
    links.append(associations);
    links.append(classDirectedAssociations);
    links.append(classCommentLinks);
    links.append(compositions);
    links.append(dependencies);
    links.append(generalizations);
    links.append(interfaceRealizations);
    links.append(realizations);


    //activity diagram
    IdList activityCommentLinks = mApi.elementsByType("ActivityDiagram_CommentLink");
    IdList activityConstraintEdges = mApi.elementsByType("ActivityDiagram_ConstraintEdge");
    IdList controlFlows = mApi.elementsByType("ActivityDiagram_ControlFlow");
    IdList objectFlows = mApi.elementsByType("ActivityDiagram_ObjectFlow");
    IdList decisionNodes = mApi.elementsByType("ActivityDiagram_DecisionNode");
    IdList mergeNodes = mApi.elementsByType("ActivityDiagram_MergeNode");

    links.append(activityCommentLinks);
    links.append(activityConstraintEdges);
    links.append(controlFlows);
    links.append(objectFlows);

    //use case diagram
    IdList useCaseCommentLinks = mApi.elementsByType("UseCaseDiagram_CommentLink");
    IdList useCaseConstraintEdges = mApi.elementsByType("UseCaseDiagram_ConstraintEdge");
    IdList useCaseDirectedAssociations = mApi.elementsByType("UseCaseDiagram_DirectedAssociation");
    IdList extends = mApi.elementsByType("UseCaseDiagram_Extend");
    IdList includes = mApi.elementsByType("UseCaseDiagram_Include");
    IdList actors = mApi.elementsByType("UseCaseDiagram_Actor");
    IdList useCases = mApi.elementsByType("UseCaseDiagram_UseCase");

    links.append(useCaseCommentLinks);
    links.append(useCaseConstraintEdges);
    links.append(useCaseDirectedAssociations);
    links.append(extends);
    links.append(includes);

    //Checking for links ends (both of them exist).
    Q_FOREACH (Id aLink, links) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId == ROOT_ID || toId == ROOT_ID) {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must have nodes on both ends.");
            result = false;
        }
    }
    //Next constraints may need links to have both: source and target.
    if (result == false) {
        return false;
    }

    //Class Methods and Class Fields are inside some Class or Interface.
    IdList features;
    features.append(classFields);
    features.append(classMethods);
    Q_FOREACH (Id id, features) {
        IdList parents = mApi.parents(id);
        if (parents.isEmpty()) {
            addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". Move it inside some Class or Interface.");
            result = false;
        }
    }

    //[Superstructure][09-02-02][1] A decision node has one or two incoming edges and at least one outgoing edge.
    Q_FOREACH (Id aDecision, decisionNodes) {
        IdList incomingLinks = mApi.incomingLinks(aDecision);
        IdList outgoingLinks = mApi.outgoingLinks(aDecision);

        if ( !(incomingLinks.size() == 2 || incomingLinks.size() == 1) || !(outgoingLinks.size() >=1) ) {
            addError("Unable to serialize object " + objectType(aDecision) + " with id: " + aDecision.toString() + ". A decision node has one or two incoming edges and at least one outgoing edge.");
            result = false;
        }
    }

    //[Superstructure][09-02-02][1] A merge node has one outgoing edge.
    Q_FOREACH (Id aMerge, mergeNodes) {
        IdList outgoingLinks = mApi.outgoingLinks(aMerge);

        if ( !outgoingLinks.size()==1 ) {
            addError("Unable to serialize object " + objectType(aMerge) + " with id: " + aMerge.toString() + ". A merge node has one outgoing edge.");
            result = false;
        }
    }

    //[Superstructure][09-02-02][3] A constraint cannot be applied to itself. (Comments too)
    IdList commentConstraintLinks;
    commentConstraintLinks.append(classCommentLinks);
    commentConstraintLinks.append(activityCommentLinks);
    commentConstraintLinks.append(activityConstraintEdges);
    commentConstraintLinks.append(useCaseCommentLinks);
    commentConstraintLinks.append(useCaseConstraintEdges);
    Q_FOREACH (Id aLink, commentConstraintLinks) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId == toId) {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It cannot be applied to itself.");
            result = false;
        }
    }

    ///[Superstructure][09-02-02][1] The source and the target of an edge must be in the same activity as the edge.
    IdList activityEdges;
    activityEdges.append(controlFlows);
    activityEdges.append(objectFlows);
    Q_FOREACH (Id aLink, activityEdges) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);
        IdList fromIdParents = mApi.parents(fromId);
        IdList toIdParents = mApi.parents(toId);

        if (fromIdParents.isEmpty() || toIdParents.isEmpty() || fromIdParents.at(0) != toIdParents.at(0)) {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". The source and the target of an edge must be in the same activity as the edge.");
            result = false;
        }
    }

    //UseCaseDiagram_Actor.
    Q_FOREACH (Id anActor, actors) {
        //Check, that there is no incoming edges.
        IdList incomingLinks = mApi.incomingLinks(anActor);
        if (!incomingLinks.isEmpty()) {
            addError("Unable to serialize object " + objectType(anActor) + " with id: " + anActor.toString() + ". An actor can not have incoming edges.");
            result = false;
        }
        IdList outgoingLinks = mApi.outgoingLinks(anActor);
        Q_FOREACH (Id aLink, outgoingLinks) {
            //Check, that just CommentLink or DirectedAssociation as an outgoing links. Not ConstrainEdge, Extend or Include.
            if (aLink.element() != "UseCaseDiagram_CommentLink" || aLink.element() != "UseCaseDiagram_DirectedAssociation") {
                addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". An actor can have only Comment Link or Directed Association as an outgoing link.");
                result = false;
            }
            //[Superstructure][09-02-02][1] An actor can only have associations to use cases [,components and classes. ...].
            Id toId = mApi.otherEntityFromLink(aLink, anActor);
            if (toId.element() != "UseCaseDiagram_UseCase") {
                addError("Unable to serialize object " + objectType(anActor) + " with id: " + anActor.toString() + ". An actor can only have associations to use cases.");
                result = false;
            }
        }

        //[Superstructure][09-02-02][2] An actor must have a name.
        if (mApi.name(anActor).simplified() == "") {
            addError("Unable to serialize object " + objectType(anActor) + " with id: " + anActor.toString() + ". An actor must have a name.");
        }
    }

    //Check, that Extend and Include connect UseCases.
    IdList extendAndIncludes;
    extendAndIncludes.append(extends);
    extendAndIncludes.append(includes);
    Q_FOREACH (Id aLink, extendAndIncludes) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId.element() != "UseCaseDiagram_UseCase" || toId.element() != "UseCaseDiagram_UseCase") {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must have Use Case on both ends.");
            result = false;
        }
    }

    //[Superstructure][09-02-02][1] A UseCase must have a name.
    Q_FOREACH (Id aUseCase, useCases) {
        if (mApi.name(aUseCase).simplified() == "") {
            addError("Unable to serialize object " + objectType(aUseCase) + " with id: " + aUseCase.toString() + ". A Use Case must have a name.");
        }
    }

    //ClassDiagram_Generalization: edge between Classes
    Q_FOREACH (Id aLink, generalizations) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId.element() != "ClassDiagram_Class" || toId.element() != "ClassDiagram_Class") {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must have Classes on both ends.");
            result = false;
        }
    }

    //ClassDiagram_InterfaceRealization: edge between Class and Interface
    Q_FOREACH (Id aLink, interfaceRealizations) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId.element() != "ClassDiagram_Class") {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must starts with a Class.");
            result = false;
        }
        if (toId.element() != "ClassDiagram_Interface") {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must ends with an Interface.");
            result = false;
        }
    }

    //ActivityDiagram_ConstraintEdge: edge with Constraint-node
    if (!commentAndConstraintChecking(activityConstraintEdges, "Activity", "Constraint")) {
        result = false;
    }

    //UseCaseDiagram_ConstraintEdge: edge with Constraint-node
    if (!commentAndConstraintChecking(useCaseConstraintEdges, "UseCase", "Constraint")) {
        result = false;
    }

    //ClassDiagram_CommentLink: edge with Comment-node
    if (!commentAndConstraintChecking(classCommentLinks, "Class", "Comment")) {
        result = false;
    }

    //ActivityDiagram_CommentLink: edge with Comment-node
    if (!commentAndConstraintChecking(activityCommentLinks, "Activity", "Comment")) {
        result = false;
    }

    //UseCaseDiagram_CommentLink: edge with Comment-node
    if (!commentAndConstraintChecking(useCaseCommentLinks, "UseCase", "Comment")) {
        result = false;
    }

    return result;
}

bool JavaHandler::commentAndConstraintChecking(IdList const &idList, QString const &diagramType, QString const &nodeType)
{
    bool result = true;
    QString fullType = diagramType + "Diagram_" +nodeType;

    Q_FOREACH (Id aLink, idList) {
        Id fromId = mApi.from(aLink);
        Id toId = mApi.to(aLink);

        if (fromId.element() != fullType && toId.element() != fullType) {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must have " + nodeType + " on one of the ends.");
            result = false;
        }
        if (fromId.element() == fullType && toId.element() == fullType) {
            addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It must have non-" + nodeType + " node on one of the ends.");
            result = false;
        }
    }

    return result;
}

Id JavaHandler::findMergeNode(Id const &idDecisionNode)
{
    Id mergeNode = Id();

    //look for Merge Nodes connected with this Decision Node
    IdList mergeNodes;
    IdList incomingConnections = mApi.incomingConnections(idDecisionNode);
    Q_FOREACH (Id aConnection, incomingConnections) {
        if (aConnection.element() == "ActivityDiagram_MergeNode") {
            mergeNodes.append(aConnection);
        }
    }

    if (mergeNodes.length() == 0) {
        mergeNode = Id();
    } else if (mergeNodes.length() == 1) {
        mergeNode = mergeNodes.at(0);
    } else {
        addError("Unable to serialize object " + objectType(idDecisionNode) + " with id: " + idDecisionNode.toString() + ". Is has to many connected Merge Nodes.");
    }

    return mergeNode;
}

Id JavaHandler::findDecisionNode(Id const &idMergeNode)
{
    Id decisionNode;

    //it is empty because there is no need in it right now, but will be in the future

    return decisionNode;
}

Id JavaHandler::findNonBodyLink(Id const &idDecisionNode)
{
    Id linkId = Id();

    IdList outgoingLinks = mApi.outgoingLinks(idDecisionNode);
    if (outgoingLinks.length() != 2) {
        addError("Unable to serialize object " + objectType(idDecisionNode) + " with id: " + idDecisionNode.toString() + ". May be you forget a Merge Node before this Decision Node.");
    } else {
        QString guard1 = getFlowGuard(outgoingLinks.at(0));
        QString guard2 = getFlowGuard(outgoingLinks.at(1));

        //they can't be equal "" at the same time; one of them ought to be "" or to be "else"
        if (guard1 != "" || guard2 != "") {
            if ( (guard1 == "" || guard1 == "else") || (guard2 == "" || guard2 == "else") ) {
                //Supposing that the first link is represent the loop's body. If not --- .swap
                if (guard1 == "" || guard1 == "else") {
                    outgoingLinks.swap(0, 1);
                }

                linkId = outgoingLinks.at(1);
            } else {
                addError("Unable to serialize object " + objectType(idDecisionNode) + " with id: " + idDecisionNode.toString() + ". One of the guards must either empty or equals \"else\".");
            }
        } else {
            addError("Unable to serialize object " + objectType(idDecisionNode) + " with id: " + idDecisionNode.toString() + ". At least one of the links must have a guard.");
        }
    }


    return linkId;
}

Id JavaHandler::findJoinNode(Id const &idForkNode)
{
    Id joinNode;

    //it is empty because there is no need in it right now, but will be in the future

    return joinNode;
}

//Returns "importaint" nodes between startNode (including) and untilNode (excluding)
IdList JavaHandler::findIntermediateNodes(Id const &startNode, Id const &untilNode, bool const closesMethod)
{
    IdList children;

    if (startNode != untilNode) {
        children.append(startNode);

        if (startNode.element() == "ActivityDiagram_DecisionNode") {
//            int isControlFlow = -1; //for checking in and outgoing links connected to DecisionNode
            if (!mApi.links(startNode).isEmpty()) {
                IdList outgoingLinks = mApi.outgoingLinks(startNode);
                if (!outgoingLinks.isEmpty()) {
//                    // Very arched way to check:
//                    // [Superstructure 09-02-02.pdf][2] The edges coming into and out of a DecisionNode, other than the decisionInputFlow (if any), must be either all ObjectFlows or all ControlFlows.
//                    Q_FOREACH (Id const aLink, outgoingLinks) {
//                        if (aLink.element() == "ActivityDiagram_ControlFlow") {
//                            if (isControlFlow == 0) {
//                                addError("Unable to serialize object " + objectType(startNode) + " with id: " + startNode.toString() + ". The edges coming out must be either all Object Flows or all Control Flows.");
//                            }
//                            isControlFlow = 1;
//                        } else if (aLink.element() == "ActivityDiagram_ObjectFlow") {
//                            if (isControlFlow == 1) {
//                                addError("Unable to serialize object " + objectType(startNode) + " with id: " + startNode.toString() + ". The edges coming out must be either all Object Flows or all Control Flows.");
//                            }
//                            isControlFlow = 0;
//                        } //TODO: To check if the link is not Control Flow or Object Flow and fail the generation if it is.
//                    }

                    //"if" or "while"?
                    IdList incomingLinks = mApi.incomingLinks(startNode);
                    deleteCommentLinks(incomingLinks);
                    deleteConstraintEdges(incomingLinks);
                    if (incomingLinks.length() == 1) { //"if"
                        Id mergeNode = findMergeNode(startNode);

                        if (mergeNode != Id()) {
                            children.append(findIntermediateNodes(mergeNode, untilNode, closesMethod));
                        }
                    } else if (incomingLinks.length() == 2) { //"while"
                        Id nonBodyLink = findNonBodyLink(startNode);

                        if (nonBodyLink != Id()) {
                            Id firstNonBodyElement = mApi.otherEntityFromLink(nonBodyLink, startNode);
                            if (firstNonBodyElement != Id()) {
                                children.append(findIntermediateNodes(firstNonBodyElement, untilNode, closesMethod));
                            } else { //wrong end of the link
                                addError("Unable to serialize object " + objectType(nonBodyLink) + " with id: " + nonBodyLink.toString() + ". A decision Node has one or two incoming edges.");
                            }
                        }
                    } else { //[Superstructure 09-02-02][1] A decision node has one or two incoming edges.
                        addError("Unable to serialize object " + objectType(startNode) + " with id: " + startNode.toString() + ". A decision Node has one or two incoming edges.");
                    }

                } else {
                    addError("Unable to serialize object " + objectType(startNode) + " with id: " + startNode.toString() + ". Is must have at least one outgoing edge.");
                }
            } else {
                addError("Unable to serialize object " + objectType(startNode) + " with id: " + startNode.toString() + ". Is must have at least one outgoing edge.");
            }
        } else if (startNode.element() == "ActivityDiagram_MergeNode") {
            //TODO: fill this
            IdList outgoingLinks = mApi.outgoingLinks(startNode);
            deleteCommentLinks(outgoingLinks);
            deleteConstraintEdges(outgoingLinks);

            Q_FOREACH (Id aLink, outgoingLinks) {
                Id nextElement = mApi.otherEntityFromLink(aLink, startNode);
                if (nextElement != Id()) {
                    children.append(findIntermediateNodes(nextElement, untilNode, closesMethod));;
                } else {
                    addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It does not have the target-node.");
                }
            }
        } else if (startNode.element() == "ActivityDiagram_ActivityFinalNode") {
            if (!closesMethod) {
                Id returnNode = children.at(children.length() - 1);
                addError("Node: " + returnNode.toString() + ". If you want \"return;\" you should write it in the Action before this Final Node.");
            }
        } else if (startNode.element() == "ActivityDiagram_Activity") { //"try-catch" or just Activity
            IdList outgoingLinks = mApi.outgoingLinks(startNode);
            deleteCommentLinks(outgoingLinks);
            deleteConstraintEdges(outgoingLinks);

            //Delete all "exception"-links from the list. They will be serialized by the Activity itself.
            Q_FOREACH (Id aLink, outgoingLinks) {
                if (getFlowGuard(aLink) != "") {
                    outgoingLinks.removeAll(aLink);
                }
            }

            Q_FOREACH (Id aLink, outgoingLinks) {
                Id nextElement = mApi.otherEntityFromLink(aLink, startNode);
                if (nextElement != Id()) {
                    children.append(findIntermediateNodes(nextElement, untilNode, closesMethod));
                } else {
                    addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It does not have the target-node.");
                }
            }
        } //TODO: add other "importaint" nodes
        else { //Think, that it is Action, Initial, Final.
            IdList outgoingLinks = mApi.outgoingLinks(startNode);
            deleteCommentLinks(outgoingLinks);
            deleteConstraintEdges(outgoingLinks);

            Q_FOREACH (Id aLink, outgoingLinks) {
                Id nextElement = mApi.otherEntityFromLink(aLink, startNode);
                if (nextElement != Id()) {
                    children.append(findIntermediateNodes(nextElement, untilNode, closesMethod));
                } else {
                    addError("Unable to serialize object " + objectType(aLink) + " with id: " + aLink.toString() + ". It does not have the target-node.");
                }
            }
        }
    }

    return children;
}

//Delete all CommentLink links from idList. Returns deleted CommentLinks.
IdList JavaHandler::deleteCommentLinks(IdList &idList)
{
    IdList result;

    Q_FOREACH (Id aLink, idList) {
        if (aLink.element() == "ActivityDiagram_CommentLink" || aLink.element() == "ClassDiagram_CommentLink"
                || aLink.element() == "UseCaseDiagram_CommentLink") {
            result.append(aLink);
            idList.removeAll(aLink);
        }
    }

    return result;
}

//Delete all ConstraintEdge links from idList. Returns deleted ConstraintEdges.
IdList JavaHandler::deleteConstraintEdges(IdList &idList)
{
    IdList result;

    Q_FOREACH (Id aLink, idList) {
        if (aLink.element() == "ActivityDiagram_ConstraintEdge") {
            result.append(aLink);
            idList.removeAll(aLink);
        }
    }

    return result;
}

//Returns "importaint" nodes between startNode (including) and untilNode (including, if it is not "Id()")
IdList JavaHandler::getActivityChildren(Id const &idStartNode, Id const &idUntilNode)
{
    IdList result;

    //if the Final Node, that we will find closes the Method
    bool closesMethod = idStartNode != Id() && idStartNode.element() == "ActivityDiagram_InitialNode";

    result.append(findIntermediateNodes(idStartNode, idUntilNode, closesMethod));
    if (idUntilNode != Id()) {
        result.append(idUntilNode);
    }

    return result;
}

QString JavaHandler::serializeChildren(Id const &idParent)
{
    QString result = "";
    IdList childElems, allChildren = mApi.children(idParent);

    if (objectType(idParent) == "ActivityDiagram_ActivityDiagramNode" || objectType(idParent) == "ActivityDiagram_Activity") {
        IdList startNodes;

        Q_FOREACH (Id aChild, allChildren) {
            //Search for the nodes that can start control flows
            //We are hoping that there is just one such node on the Activity Diagram
            if (aChild.element() == "ActivityDiagram_InitialNode" || aChild.element() == "ActivityDiagram_ActivityParameterNode"
                || aChild.element() == "ActivityDiagram_AcceptEventAction") {
                startNodes.append(aChild);
            }
        }

        if (startNodes.length() == 0) {
            addError("Unable to serialize object " + objectType(idParent) + " with id: " + idParent.toString() + ". There is no start nodes (Initial Node, Activity Parameter Node, Accept Event Action).");
        } else if (startNodes.length() > 1) { //TODO: to find out if it is possible. If so, write the appropriate serialization
            addError("AAA!!! There are too many start nodes. I don't know which to choose.");
        } else {
            Id startNode = startNodes.at(0);
            childElems = getActivityChildren(startNode, Id());
        }
    } else if (objectType(idParent) == "ClassDiagram_Class") {
        //All fields in the beginning, all methods in the end.
        IdList fields, methods;

        Q_FOREACH (Id aChild, allChildren) {
            if (aChild.element() == "ClassDiagram_ClassField") {
                fields.append(aChild);
            } else if (aChild.element() == "ClassDiagram_ClassMethod") {
                methods.append(aChild);
            } //TODO: add processing of another nodes that may be in Class
        }

        childElems.append(fields);
        childElems.append(methods);
    } else {
        childElems = allChildren;
    }


    Q_FOREACH (Id const id, childElems) {
        result += serializeObject(id);
    }

    return result;
}

QString JavaHandler::serializeObject(Id const &id)
{
    QString result = "";

    // class diagram

    if ((objectType(id) == "ClassDiagram_Class")||(objectType(id) == "ClassDiagram_Interface")) {

        mIndent = 0;

        //-----------
        QString const pathToFile = pathToDir + "/" + mApi.name(id) + ".java";

        QFile file(pathToFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return "";

        QTextStream out(&file);
        //-----------

        QString visibility = getVisibility(id);
        QString multiplicity = getMultiplicity(id);
        QString imports = getImports(id);
        QString isFinalField = hasModifier(id, "final");
        QString isAbstractField = hasModifier(id, "abstract");
        QString parents = getSuperclass(id);
        QString interfaces = getInterfaces(id);

        if (isAbstractField == "abstract " && isFinalField == "final ") {
            addError("unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". \"abstract final\" declaration doesn't make sense");
        }

        out << getComments(id);
        out << imports;
        out << indent() + visibility + isAbstractField + isFinalField + "class " + mApi.name(id) + parents + interfaces +  " {" + "\n";
        mIndent++;

        if (!mApi.links(id).isEmpty()) {
            // search for the Class-typed attrbutes
            IdList outgoingLinks = mApi.outgoingLinks(id);
            Q_FOREACH (Id const aLink, outgoingLinks) {
                if (aLink.element() == "ClassDiagram_DirectedAssociation" || aLink.element() == "ClassDiagram_Association"
                        || aLink.element() == "ClassDiagram_Composition" || aLink.element() == "ClassDiagram_Aggregation") {
                    QString type = mApi.name(mApi.otherEntityFromLink(aLink, id)) + " ";
                    QString visibility = getVisibility(aLink);
                    QString isFinalField = hasModifier(aLink, "final");
                    QString isStaticField = hasModifier(aLink, "static");
                    QString isVolatileField = hasModifier(aLink, "volatile");
                    QString isTransientField = hasModifier(aLink, "transient");

                    if (isVolatileField == "volatile " && isFinalField == "final ") {
                        addError("unable to serialize object " + objectType(id) + " with id: " + aLink.toString() + ". \"final volatile\" declaration doesn't make sense");
                    }

                    out << indent() + isFinalField + visibility + isStaticField + isVolatileField + isTransientField + type + mApi.name(aLink);
                    out << ";\n";
                }
            }

            //search for bidirectional assocciation
            IdList linksIn = mApi.incomingLinks(id);
            Q_FOREACH (Id const aLink, linksIn) {
                if (aLink.element() == "ClassDiagram_Association" || aLink.element() == "ClassDiagram_Composition"
                        || aLink.element() == "ClassDiagram_Aggregation") {
                    QString type = mApi.name(mApi.otherEntityFromLink(aLink, id)) + " ";
                    QString visibility = getVisibility(aLink);
                    QString isFinalField = hasModifier(aLink, "final");
                    QString isStaticField = hasModifier(aLink, "static");
                    QString isVolatileField = hasModifier(aLink, "volatile");
                    QString isTransientField = hasModifier(aLink, "transient");

                    if (isVolatileField == "volatile " && isFinalField == "final ") {
                        addError("unable to serialize object " + objectType(id) + " with id: " + aLink.toString() + ". \"final volatile\" declaration doesn't make sense");
                    }

                    out << indent() + isFinalField + visibility + isStaticField + isVolatileField + isTransientField + type + mApi.name(aLink);
                    out << ";\n";
                }
            }
        }

        out << serializeChildren(id);
        mIndent--;
        out << indent() + "} //end of class\n";
    } else if (objectType(id) == "ClassDiagram_View") {
        //	    to do someting
    } else if (objectType(id) == "ClassDiagram_ClassMethod") {
        IdList parents = mApi.parents(id);
        if (!parents.isEmpty()) {
            Id parentId = parents.at(0);
            QString const parentType = objectType(parentId);

            if (parentType == "ClassDiagram_Class" || parentType == "ClassDiagram_Interface") {
                QString visibility = getVisibility(id);
                QString type = getType(id);
                QString operationFactors = getOperationFactors(id);
                QString isFinalField = hasModifier(id, "final");
                QString isAbstractField = hasModifier(id, "abstract");
                QString isStaticField = hasModifier(id, "static");
                QString isSynchronizedField = hasModifier(id, "synchronized");
                QString isNativeField = hasModifier(id, "native");

                if (isAbstractField == "abstract ") {
                    if (isFinalField == "final ") {
                        addError("unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". \"abstract final\" declaration doesn't make sense");
                    } else if (isStaticField == "static ") {
                        addError("unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". \"abstract static\" declaration doesn't make sense");
                    }
                }

                QString methodBody = getMethodCode(id);

                result += getComments(id);
                result += indent() + visibility + isAbstractField + isStaticField + isFinalField + isSynchronizedField + isNativeField +
                          type  + mApi.name(id) + "(" + operationFactors + ") " + methodBody + "\n";
            } else {
                addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". Move it inside some Class or Interface.");
            }
        }
    } else if (objectType(id) == "ClassDiagram_ClassField") {
        IdList parents = mApi.parents(id);
        if (!parents.isEmpty()) {
            Id parentId = parents.at(0);
            QString const parentType = objectType(parentId);

            if (parentType == "ClassDiagram_Class" || parentType == "ClassDiagram_Interface") {
                QString visibility = getVisibility(id);
                QString type = getType(id);
                QString defaultValue = getDefaultValue(id);
                QString isFinalField = hasModifier(id, "final");
                QString isStaticField = hasModifier(id, "static");
                QString isVolatileField = hasModifier(id, "volatile");
                QString isTransientField = hasModifier(id, "transient");

                if (isVolatileField == "volatile " && isFinalField == "final ") {
                    addError("unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". \"final volatile\" declaration doesn't make sense");
                }

                result += getComments(id);
                result += indent() +  visibility + isStaticField + isFinalField + isVolatileField + isTransientField + type + mApi.name(id);
                if (defaultValue != "") {
                    result += " = " + defaultValue;
                }
                result += ";\n";
            } else {
                addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". Move it inside some Class or Interface.");
            }
        }
    }

    // activity diagram

    else if (objectType(id) == "ActivityDiagram_InitialNode") {
        //[Superstructure 09-02-02][1] An initial node has no incoming edges.
        if (!mApi.links(id).isEmpty()) {
            if (!mApi.incomingLinks(id).isEmpty()) {
                addError("Object " + objectType(id) + " with id  " + id.toString() + " can not have incoming links.");
            }

            //[Superstructure 09-02-02][2] Only control edges can have initial nodes as source.
            IdList outgoingLinks = mApi.outgoingLinks(id);
            Q_FOREACH (Id const aLink, outgoingLinks) {
                if (aLink.element() == "ActivityDiagram_ObjectFlow") {
                    addError("Object " + objectType(id) + " with id  " + id.toString() + ". Only Control Edges can have Initial Nodes as source.");
                }
            }
        }

        result += getConstraints(id);
    } else if (objectType(id) == "ActivityDiagram_Action") {
        IdList outgoingConnections = mApi.outgoingConnections(id);
        IdList activityDiagrams;
        Q_FOREACH (Id aConnection, outgoingConnections) {
            if (aConnection.element() == "ActivityDiagram_ActivityDiagramNode") {
                activityDiagrams.append(aConnection);
            }
        }

        if (activityDiagrams.length() > 1) {
            addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". There are too many Activity Diagrams connected.");
        }

        result += getComments(id);
        QString code = getConstraints(id);

        if (activityDiagrams.isEmpty() && code == "") {
            result += indent() + mApi.name(id).replace("<br/>", "\n" + indent()) + "\n";
        } else {
            if (activityDiagrams.isEmpty() && code != "") {
                result += code;
            } else if (!activityDiagrams.isEmpty() && code == "") {
                result += serializeChildren(activityDiagrams.at(0));
            } else {
                addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". You can not set code into the Constraints and Activity Diagram at the same time.");
            }
        }
    } else if (objectType(id) == "ActivityDiagram_ActivityFinalNode") {
        result += getComments(id) + getConstraints(id);

        //[Superstructure 09-02-02][1] A final node has no outgoing edges.
        IdList outgoingLinks = mApi.outgoingLinks(id);
        if (!outgoingLinks.isEmpty()) {
            addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". A final node has no outgoing edges.");
        }
    } else if (objectType(id) == "ActivityDiagram_DecisionNode") {
        result += getComments(id);
        //"if" or "while"?
        IdList incomingLinks = mApi.incomingLinks(id);
        deleteCommentLinks(incomingLinks);
        deleteConstraintEdges(incomingLinks);
        if (incomingLinks.length() == 1) { //"if"
            result += ifStatement(id);
        } else if (incomingLinks.length() == 2) { //"while"
            result += whileDoLoop(id);
        }

        result += indent() + "\n";
    } else if (objectType(id) == "ActivityDiagram_MergeNode") {
        result += getComments(id);
    } else if (objectType(id) == "ActivityDiagram_Activity") {
        result += getComments(id) + getConstraints(id);
        //search for "exception"-links
        IdList outgoingLinks = mApi.outgoingLinks(id);
        IdList exceptions;
        Q_FOREACH (Id aLink, outgoingLinks) {
            if (getFlowGuard(aLink) != "") {
                exceptions.append(aLink);
            }
        }

        if (exceptions.isEmpty()) { //if it is just an Activity
            result += serializeChildren(id);
        } else { //if it is "try-catch"
            result += tryCatch(id);
        }
    }

    return result;
}

QString JavaHandler::serializeActivity(Id const &idStartNode, Id const &idUntilNode)
{
    QString result = "";

    IdList childElems = getActivityChildren(idStartNode, idUntilNode);

    if (!childElems.isEmpty()) {
        Id last = childElems.takeLast(); //it will be serialized in the upper level activity

        if (last != Id() && (last.element() == "ActivityDiagram_Action" ||
                             last.element() == "ActivityDiagram_Activity")) {
            childElems.append(last);
        }
    }

    Q_FOREACH (Id id, childElems) {
       if (id != Id()) {
            result += serializeObject(id);
        }
    }

    return result;
}

QString JavaHandler::tryCatch(Id const &id)
{
    QString result = "";
    int existFinally = 0; //for checking that there is no 2 outgoing links with "finally" as a guard

    //search for "exception"-links
    IdList outgoingLinks = mApi.outgoingLinks(id);
    IdList exceptions;
    Q_FOREACH (Id aLink, outgoingLinks) {
        if (getFlowGuard(aLink) != "") {
            exceptions.append(aLink);
        }
    }

    //move "finally"-link to the end of the list
    Q_FOREACH (Id aLink, exceptions) {
        //if this link represent "finally" case than change it with the last link in the serialization sequence
        if (getFlowGuard(aLink) == "finally") {
            if (existFinally == 1) {
                addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". There are two objects with \"finally\" as guard.");
            }
            existFinally = 1;
            outgoingLinks.swap(outgoingLinks.indexOf(aLink), outgoingLinks.length()-1);
        }
    }

    result += indent() + "try {\n";
    mIndent++;
    result += serializeChildren(id);
    mIndent--;
    result += indent() + "}";

    Q_FOREACH (Id anException, exceptions) {
        if (getFlowGuard(anException) != "finally") {
            result += " catch (" + getFlowGuard(anException) + ") {\n";
        } else {
            result += " finally {\n";
        }
        mIndent++;
        Id exceptionHandler = mApi.otherEntityFromLink(anException, id);
        result += serializeActivity(exceptionHandler, Id());
        mIndent--;
        result += indent() + "}";
    }

    result += "\n";

    return result;
}

QString JavaHandler::ifStatement(Id const &id)
{
    QString result = indent() + "";

    Id untilMergeNode = findMergeNode(id);
    int existElse = 0; //for checking that there is no 2 outgoing links with "else" as a guard

    //move "else" link to the end of the list and delete Constraint Edges.
    IdList outgoingLinks = mApi.outgoingLinks(id);
    deleteCommentLinks(outgoingLinks);
    deleteConstraintEdges(outgoingLinks);
    Q_FOREACH (Id aLink, outgoingLinks) {
        if (aLink.element() == "ActivityDiagram_ConstraintEdge") {
            outgoingLinks.removeAll(aLink);
            break;
        }
        //if this link represent "else" case than change it with the last link in the serialization sequence
        if (getFlowGuard(aLink) == "else" || getFlowGuard(aLink) == "") {
            if (existElse == 1) {
                addError("Unable to serialize object " + objectType(id) + " with id: " + id.toString() + ". There are two objects with \"else\" as guard.");
            }
            existElse = 1;
            outgoingLinks.swap(outgoingLinks.indexOf(aLink), outgoingLinks.length()-1);
        }
    }

    //serialization
    Q_FOREACH (Id aLink, outgoingLinks) {
        Id caseBody = mApi.otherEntityFromLink(aLink, id);
        if (caseBody != untilMergeNode) {
            if (aLink != outgoingLinks.at(0)) { //if it is not the first link, connected to the Decision Node
                result += " else ";
            }

            QString guard = getFlowGuard(aLink);
            if (guard != "else" && guard != "") {
                result += "if (" + guard + ") ";
            }
            result += "{\n";
            mIndent++;
            result += serializeActivity(caseBody, untilMergeNode);
            mIndent--;
            result += indent() + "}";

        }
    }

    //if there is no merge node than we must close the function
    if (untilMergeNode == Id()) {
        mIndent--;
        result += indent() + "}\n";
    }

    return result;
}

QString JavaHandler::whileDoLoop(Id const &id)
{
    QString result = indent() + "";

    //get the "body" link
    Id nonBodyLink = findNonBodyLink(id);
    IdList outgoingLinks = mApi.outgoingLinks(id);
    outgoingLinks.removeAll(nonBodyLink);
    Id bodyLink = outgoingLinks.at(0);

    result += "while (" + getFlowGuard(bodyLink) + ") {\n";
    mIndent++;

    //Serialization of the loop's body
    Id nextElement = mApi.otherEntityFromLink(bodyLink, id);
    result += serializeActivity(nextElement, id);
\
    mIndent--;
    result += indent() + "}\n";

    return result;
}

QString JavaHandler::doWhileLoop(Id const &id)
{
    QString result = "";

    return result;
}

QString JavaHandler::getVisibility(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "visibility")) {
        QString visibility = mApi.stringProperty(id, "visibility");

        if (visibility == "+")
            visibility = "public";
        else if (visibility == "-")
            visibility = "private";
        else if (visibility == "#")
            visibility = "protected";
        else if (visibility == "~")
            visibility = "";

        if (isVisibilitySuitable(visibility)) {
            result = visibility;
            if (result != "")
                result += " ";
        } else {
            addError("Object " + objectType(id) + " with id  " + id.toString() + " has invalid visibility property: " + visibility);
        }
    }

    return result;
}

QString JavaHandler::getFlowGuard(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "guard")) {
        QString guard = mApi.stringProperty(id, "guard");
        //TODO: delete it!!! That is just because QReal's sh~
        guard.replace("&lt;", "<");
        guard.replace("&rt;", ">");

        result = guard.simplified(); //delete whitespaces from the start and the end and internal whitespaces replace with a single space

//        if (guard == "") {
//            addError("Object " + objectType(id) + " with id  " + id.toString() + " has empty guard property.");
//        }
    }

    return result;
}

QString JavaHandler::getMultiplicity(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "multiplicity")) {
        QString multiplicity = mApi.stringProperty(id, "multiplicity");
        result = multiplicity;
    }

    return result;
}

QString JavaHandler::getType(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "type")) {
        QString type = mApi.stringProperty(id, "type");

        result = type + " ";
        if ( !isTypeSuitable(type) && !(objectType(id) == "ClassDiagram_ClassMethod" && type == "void") ) {
            addError("Object " + objectType(id) + " with id " + id.toString() + " has invalid type: " + type);
        }
    }

    return result;
}

QString JavaHandler::getSuperclass(Id const &id)
{
    QString result = "";
    bool hasParentClass = false;

    if (!mApi.links(id).isEmpty()) {
        IdList links = mApi.outgoingLinks(id);

        Q_FOREACH (Id const aLink, links) {
            if (aLink.element() == "ClassDiagram_Generalization") {
                if (hasParentClass == false) {
                    hasParentClass = true;
                    if (id == mApi.otherEntityFromLink(aLink, id)) {
                        addError("Object " + objectType(id) + " with id " + id.toString() + " can not be his own superclass");
                    } else {
                        QString parentClass = mApi.name(mApi.otherEntityFromLink(aLink, id));
                        result += " extends " + parentClass;
                    }
                } else {
                    addError("Object " + objectType(id) + " with id " + id.toString() + " has too many superclasses");
                }
            }
        }

    }

    return result;
}

QString JavaHandler::getInterfaces(Id const &id)
{
    QString result = "";
    bool hasInterface = false;

    if (!mApi.links(id).isEmpty()) {
        IdList links = mApi.outgoingLinks(id);

        Q_FOREACH (Id const aLink, links) {
            if (aLink.element() == "ClassDiagram_InterfaceRealization") {
                 QString interface = mApi.name(mApi.otherEntityFromLink(aLink, id));
                 hasInterface = true;
                 if (!hasInterface) {
                     result += " implements " + interface;
                 } else {
                     result += ", " + interface;
                 }
            }
        }

    }

    return result;
}

QString JavaHandler::getMethodCode(Id const &id)
{
    QString result = "{\n";
    mIndent++;

    if (!mApi.outgoingConnections(id).isEmpty()) {
        IdList outgoingConnections = mApi.outgoingConnections(id);
        Id realizationDiagram;

        int realizationsCount = 0;
        Q_FOREACH (Id aConnection, outgoingConnections) {
            if (aConnection.element() == "ActivityDiagram_ActivityDiagramNode") {
                realizationDiagram = aConnection;
                realizationsCount++;
            }
        }

        if (realizationsCount == 1) { //if everything is ok
            result += serializeChildren(realizationDiagram);
        } else if (realizationsCount == 0) { //if there is no realization
            addError("Method " + objectType(id) + " with id " + id.toString() + " will be empty.");
        } else { //if there is more than one ActivityDiagram connected
            addError("Object " + objectType(id) + " with id " + id.toString() + " has too many realizations");
        }

    } else {
        addError("Method " + objectType(id) + " with id " + id.toString() + " will be empty.");
    }

    mIndent--;
    result += indent() + "}\n";
    return result;
}

QString JavaHandler::hasModifier(Id const &id, QString const &modifier)
{
    QString result = "";

    QString isModifier = "";

    if (modifier == "final") {
        isModifier = "isLeaf";
    } else {
        isModifier = "is" + modifier.left(1).toUpper() + modifier.mid(1, modifier.length());
    }

    if (mApi.hasProperty(id, isModifier)) {
        QString hasModifier = mApi.stringProperty(id, isModifier);

        if (hasModifier == "true") {
            result = modifier + " ";
        }else if (hasModifier != "false" && hasModifier != "") {
            addError("Object " + objectType(id) + " with id " + id.toString() + " has invalid " + isModifier + " value: " + hasModifier);
        }
    }

    return result;
}

QString JavaHandler::getOperationFactors(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "type")) {
        QString operationFactors = mApi.stringProperty(id, "operationFactors");

        result = operationFactors;
    }

    return result;
}

QString JavaHandler::getDefaultValue(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "defaultValue")) {
        QString defaultValue = mApi.stringProperty(id, "defaultValue");

        result = defaultValue;
    }

    return result;
}

QString JavaHandler::getImports(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "elementImport")) {
        QString elementImport = mApi.stringProperty(id, "elementImport");
        if (elementImport != "") {
            QStringList imports = elementImport.split(",");

            Q_FOREACH (QString anImport, imports) {
                anImport = anImport.simplified();
                if (anImport != "")
                    result += indent() + "import " + anImport + ";\n";
            }
            result += indent() + "\n";
        }
    }

    return result;
}

QString JavaHandler::getConstraints(Id const &id)
{
    QString result = "";

    //if it has the Constraint nodes
    IdList outgoingLinks = mApi.outgoingLinks(id);
    IdList constraints = deleteConstraintEdges(outgoingLinks);
    IdList incomingLinks = mApi.incomingLinks(id);
    constraints.append(deleteConstraintEdges(incomingLinks));

    if (constraints.length() >= 2) {
        addError("Object " + objectType(id) + " with id: " + id.toString() + ". The order of Constraints generalization can not be defined.");
    }

    Q_FOREACH (Id aConstraint, constraints) {
        Id constraint = mApi.otherEntityFromLink(aConstraint, id);
        result += getConstraint(constraint) + "\n";
    }

    return result;
}

QString JavaHandler::getConstraint(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "specification")) {
        QString specification = mApi.stringProperty(id, "specification");
        specification.replace("<br/>", "\n" + indent());

        result = indent() + specification;
    }

    return result;
}

QString JavaHandler::getComments(Id const &id)
{
    QString result = "";

    IdList outgoingLinks = mApi.outgoingLinks(id);
    IdList comments = deleteCommentLinks(outgoingLinks);
    IdList incominglinks = mApi.incomingLinks(id);
    comments.append(deleteCommentLinks(incominglinks));

    if (comments.length() >= 2) {
        addError("Object " + objectType(id) + " with id: " + id.toString() + ". The order of Comments generalization can not be defined.");
    }

    Q_FOREACH (Id aCommentLink, comments) {
        Id comment = mApi.otherEntityFromLink(aCommentLink, id);
        result += getComment(comment) + "\n";
    }

    return result;
}

QString JavaHandler::getComment(Id const &id)
{
    QString result = "";

    if (mApi.hasProperty(id, "body")) {
        QString body = mApi.stringProperty(id, "body");

        QStringList strings = body.split("<br/>");
        if (strings.length() == 1) {
            result = indent() + "// " + body;
        } else {
            result += indent() + "/*\n";
            Q_FOREACH (QString aString, strings) {
                result += indent() + "* " + aString + "\n";
            }
            result += indent() + "*/";
        }
    }

    return result;
}

QString JavaHandler::objectType(Id const &id)
{
    return mApi.typeName(id);
}

bool JavaHandler::isTypeSuitable(QString const &type) const
{
    //  todo: check class-type for correctness
    return type == "int" || type == "float" || type == "double" || type == "boolean"
            || type == "char" || type == "byte" || type == "long" || type == "short";
}

bool JavaHandler::isVisibilitySuitable(QString const &visibility) const
{
    return visibility == "public" || visibility == "private" || visibility == "protected" || visibility == "";
}

void JavaHandler::addError(QString const &errorText)
{
    mErrorText += errorText + "\n";
}

QString JavaHandler::indent()
{
    QString result;

    for (int i = 0; i < mIndent; ++i)
        result += "\t";
    return result;
}
